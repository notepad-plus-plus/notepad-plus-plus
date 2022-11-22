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

#define PM_PROJECTPANELTITLE     TEXT("Project Panel")
#define PM_WORKSPACEROOTNAME     TEXT("Workspace")
#define PM_NEWFOLDERNAME         TEXT("Folder Name")
#define PM_NEWPROJECTNAME        TEXT("Project Name")

#define PM_NEWWORKSPACE            TEXT("New Workspace")
#define PM_OPENWORKSPACE           TEXT("Open Workspace")
#define PM_RELOADWORKSPACE         TEXT("Reload Workspace")
#define PM_SAVEWORKSPACE           TEXT("Save")
#define PM_SAVEASWORKSPACE         TEXT("Save As...")
#define PM_SAVEACOPYASWORKSPACE    TEXT("Save a Copy As...")
#define PM_NEWPROJECTWORKSPACE     TEXT("Add New Project")
#define PM_FINDINFILESWORKSPACE    TEXT("Find in Projects...")

#define PM_EDITRENAME              TEXT("Rename")
#define PM_EDITNEWFOLDER           TEXT("Add Folder")
#define PM_EDITADDFILES            TEXT("Add Files...")
#define PM_EDITADDFILESRECUSIVELY  TEXT("Add Files from Directory...")
#define PM_EDITREMOVE              TEXT("Remove\tDEL")
#define PM_EDITMODIFYFILE          TEXT("Modify File Path")

#define PM_WORKSPACEMENUENTRY      TEXT("Workspace")
#define PM_EDITMENUENTRY           TEXT("Edit")

#define PM_MOVEUPENTRY             TEXT("Move Up\tCtrl+Up")
#define PM_MOVEDOWNENTRY           TEXT("Move Down\tCtrl+Down")

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

	virtual void display(bool toShow = true) const {
		DockingDlgInterface::display(toShow);
	};

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};

	void setPanelTitle(generic_string title) {
		_panelTitle = title;
	};
	const TCHAR * getPanelTitle() const {
		return _panelTitle.c_str();
	};

	void newWorkSpace();
	bool saveWorkspaceRequest();
	bool openWorkSpace(const TCHAR *projectFileName, bool force = false);
	bool saveWorkSpace();
	bool saveWorkSpaceAs(bool saveCopyAs);
	void setWorkSpaceFilePath(const TCHAR *projectFileName){
		_workSpaceFilePath = projectFileName;
	};
	const TCHAR * getWorkSpaceFilePath() const {
		return _workSpaceFilePath.c_str();
	};
	bool isDirty() const {
		return _isDirty;
	};
	bool checkIfNeedSave();

	virtual void setBackgroundColor(COLORREF bgColour) {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
	};
	virtual void setForegroundColor(COLORREF fgColour) {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
	};
	bool enumWorkSpaceFiles(HTREEITEM tvFrom, const std::vector<generic_string> & patterns, std::vector<generic_string> & fileNames);

protected:
	TreeView _treeView;
	HIMAGELIST _hImaLst = nullptr;
	HWND _hToolbarMenu = nullptr;
	HMENU _hWorkSpaceMenu = nullptr;
	HMENU _hProjectMenu = nullptr;
	HMENU _hFolderMenu = nullptr;
	HMENU _hFileMenu = nullptr;
	generic_string _panelTitle;
	generic_string _workSpaceFilePath;
	generic_string _selDirOfFilesFromDirDlg;
	bool _isDirty = false;
	int _panelID = 0;

	void initMenus();
	void destroyMenus();
	void addFiles(HTREEITEM hTreeItem);
	void addFilesFromDirectory(HTREEITEM hTreeItem);
	void recursiveAddFilesFrom(const TCHAR *folderPath, HTREEITEM hTreeItem);
	HTREEITEM addFolder(HTREEITEM hTreeItem, const TCHAR *folderName);

	bool writeWorkSpace(const TCHAR *projectFileName = NULL);
	generic_string getRelativePath(const generic_string & fn, const TCHAR *workSpaceFileName);
	void buildProjectXml(TiXmlNode *root, HTREEITEM hItem, const TCHAR* fn2write);
	NodeType getNodeType(HTREEITEM hItem);
	void setWorkSpaceDirty(bool isDirty);
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplayPoint(int iButton);
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void showContextMenuFromMenuKey(HTREEITEM selectedItem, int x, int y);
	HMENU getMenuHandler(HTREEITEM selectedItem);
	generic_string getAbsoluteFilePath(const TCHAR * relativePath);
	void openSelectFile();
	void setFileExtFilter(CustomFileDialog & fDlg);
	std::vector<generic_string*> fullPathStrs;
};

class FileRelocalizerDlg : public StaticDialog
{
public :
	FileRelocalizerDlg() = default;
	void init(HINSTANCE hInst, HWND parent) {
		Window::init(hInst, parent);
	};

	int doDialog(const TCHAR *fn, bool isRTL = false);

	virtual void destroy() {
	};

	generic_string getFullFilePath() {
		return _fullFilePath;
	};

protected :
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
	generic_string _fullFilePath;

};
