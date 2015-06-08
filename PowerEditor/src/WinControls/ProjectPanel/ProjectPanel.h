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


#ifndef PROJECTPANEL_H
#define  PROJECTPANEL_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "TreeView.h"
#include "ProjectPanel_rc.h"
#include "Directory.h"
#include "DirectoryWatcher.h"
#include <map>

#define PM_PROJECTPANELTITLE     TEXT("Project")
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

#define PM_EDITRENAME              TEXT("Rename")
#define PM_EDITNEWFOLDER           TEXT("Add Folder")
#define PM_EDITADDFILES            TEXT("Add Files...")
#define PM_EDITADDFILESRECUSIVELY  TEXT("Add Files from Directory...")
#define PM_EDITADDFOLDERMONITOR    TEXT("Add Folder Monitor...")
#define PM_EDITREMOVE              TEXT("Remove\tDEL")
#define PM_EDITMODIFYFILE          TEXT("Modify File Path")

#define PM_WORKSPACEMENUENTRY      TEXT("Workspace")
#define PM_EDITMENUENTRY           TEXT("Edit")

#define PM_MOVEUPENTRY             TEXT("Move Up\tCtrl+Up")
#define PM_MOVEDOWNENTRY           TEXT("Move Down\tCtrl+Down")

enum NodeType {
	nodeType_dummy = -1, nodeType_root = 0, nodeType_project = 1, nodeType_folder = 2, nodeType_file = 3, nodeType_monitorFolderRoot = 4, nodeType_monitorFolder = 5, nodeType_monitorFile = 6,
};

class TiXmlNode;

class ProjectPanelData : public TreeViewData {
public:
	generic_string _name;
	generic_string _filePath;
	NodeType _nodeType;
	DirectoryWatcher* _directoryWatcher;
	HTREEITEM _hItem;
	bool _watch;

	ProjectPanelData(DirectoryWatcher* directoryWatcher, const TCHAR* name, const TCHAR* filePath, NodeType nodeType) 
		: TreeViewData()
		, _name(name)
		, _nodeType(nodeType)
		, _directoryWatcher(directoryWatcher)
		, _hItem(NULL)
		, _watch(false)
	{
		if (filePath != NULL)
			_filePath = filePath;
	}

	virtual ~ProjectPanelData() {
		setItem(NULL);
	}

	void setItem(HTREEITEM hItem)
	{
		if (_hItem && _hItem != hItem && _watch)
			watchDir(false);
		_hItem = hItem;
	}

	bool watchDir(bool watch)
	{
		if (_hItem && _watch)
			if (_nodeType == nodeType_monitorFolderRoot || _nodeType == nodeType_monitorFolder)
				_directoryWatcher->removeDir(_filePath,_hItem);

		_watch = watch;
		if( !watch)
			return true;

		if (_hItem)
			if (watch && (_nodeType == nodeType_monitorFolderRoot || _nodeType == nodeType_monitorFolder))
				_directoryWatcher->addDir(_filePath,_hItem);
			else
				return true;

		return !(_nodeType == nodeType_monitorFolderRoot || _nodeType == nodeType_monitorFolder);
	}

	bool isRoot() const {
		return _nodeType == nodeType_root;
	}
	bool isProject() const {
		return _nodeType == nodeType_project;
	}
	bool isFile() const {
		return _nodeType == nodeType_file;
	}
	bool isFolder() const {
		return _nodeType == nodeType_folder;
	}
	bool isFileMonitor() const {
		return _nodeType == nodeType_monitorFile;
	}
	bool isFolderMonitor() const {
		return _nodeType == nodeType_monitorFolder;
	}
	bool isFolderMonitorRoot() const {
		return _nodeType == nodeType_monitorFolderRoot;
	}
	bool isDummy() const {
		return _nodeType == nodeType_dummy;
	}

	virtual TreeViewData* clone() const {
		return new ProjectPanelData(_directoryWatcher, _name.c_str(), _filePath.c_str(), _nodeType);
	}

private:
	ProjectPanelData(const ProjectPanelData&) {}
	ProjectPanelData& operator= (const ProjectPanelData&) {}
};




class ProjectPanel;

class ProjectPanelDirectory : public Directory {
protected:
	ProjectPanel* _projectPanel;
	TreeView* _treeView;
	HTREEITEM _hItem;
	std::map<generic_string,HTREEITEM> _dirMap;
	std::map<generic_string,HTREEITEM> _fileMap;
	bool _wasInitiallyEmpty;

public:
	ProjectPanelDirectory( ProjectPanel *projectPanel, HTREEITEM hItem );
	virtual ~ProjectPanelDirectory() {}

protected:

	virtual void onBeginSynchronize(const Directory& other);
	virtual void onDirAdded(const generic_string& name);
	virtual void onDirRemoved(const generic_string& name);
	virtual void onFileAdded(const generic_string& name);
	virtual void onFileRemoved(const generic_string& name);

};




class ProjectPanel : public DockingDlgInterface, public TreeViewListener {
	friend class ProjectPanelDirectory;

public:
	ProjectPanel()
		: DockingDlgInterface(IDD_PROJECTPANEL)
		, _treeView()
		, _hToolbarMenu(NULL)
		, _hWorkSpaceMenu(NULL)
		, _hProjectMenu(NULL)
		, _hFolderMenu(NULL)
		, _hFileMenu(NULL)
		, _hFolderMonitorMenu(NULL)
		, _directoryWatcher(NULL)
	{
		_treeView.setListener(this);
	};

	virtual ~ProjectPanel() {
		_treeView.setListener(NULL);
		delete _directoryWatcher;
	}


	void init(HINSTANCE hInst, HWND hPere) {
		DockingDlgInterface::init(hInst, hPere);
	}

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	void newWorkSpace();
	bool openWorkSpace(const TCHAR *projectFileName);
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
	void checkIfNeedSave(const TCHAR *title);

	virtual void setBackgroundColor(COLORREF bgColour) {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
    };
	virtual void setForegroundColor(COLORREF fgColour) {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
    };

	TreeView& getTreeView() { return _treeView; }

protected:
	TreeView _treeView;
	HIMAGELIST _hImaLst;
	HWND _hToolbarMenu;
	HMENU _hWorkSpaceMenu, _hProjectMenu, _hFolderMenu, _hFileMenu, _hFolderMonitorMenu;
	generic_string _workSpaceFilePath;
	generic_string _selDirOfFilesFromDirDlg;
	bool _isDirty;
	DirectoryWatcher* _directoryWatcher;

	void initMenus();
	void destroyMenus();
	BOOL setImageList(int root_clean_id, int root_dirty_id, int project_id, int open_node_id, int closed_node_id, int leaf_id, int ivalid_leaf_id, int open_monitor_id, int closed_monitor_id, int invalid_monitor_id, int file_monitor_id);
	void addFiles(HTREEITEM hTreeItem);
	void addFilesFromDirectory(HTREEITEM hTreeItem, bool monitored);
	void recursiveAddFilesFrom(const TCHAR *folderPath, HTREEITEM hTreeItem, bool monitored, bool recursive);
	HTREEITEM addFolder(HTREEITEM hTreeItem, const TCHAR *folderName, bool monitored = false, bool root = false, const TCHAR *monitorPath = NULL, bool sortIn = false );

	bool writeWorkSpace(TCHAR *projectFileName = NULL);
	generic_string getRelativePath(const generic_string & fn, const TCHAR *workSpaceFileName);
	void buildProjectXml(TiXmlNode *root, HTREEITEM hItem, const TCHAR* fn2write);
	NodeType getNodeType(HTREEITEM hItem) const;
	void setWorkSpaceDirty(bool isDirty);
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplyPoint(int iButton);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem);
	void rebuildFolderMonitorTree(HTREEITEM hParentItem, const ProjectPanelData& data);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	generic_string getAbsoluteFilePath(const TCHAR * relativePath);
	void openSelectFile();
	HMENU getContextMenu(HTREEITEM hTreeItem) const;
	void expandOrCollapseMonitorFolder(bool expand, HTREEITEM hItem);

	void removeDummies(HTREEITEM hTreeItem);

	void treeItemChanged(HTREEITEM hItem,TreeViewData* data);

	// TreeViewListener
	virtual void onTreeItemAdded(bool afterClone, HTREEITEM hItem, TreeViewData* newData);
	virtual void onMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	//

	static ProjectPanelData* getData(TreeViewData* data) {
		return (ProjectPanelData*) data;
	}
	static const ProjectPanelData* getData( const TreeViewData* data ) {
		return (ProjectPanelData*) data;
	}
};

class FileRelocalizerDlg : public StaticDialog
{
public :
	FileRelocalizerDlg() : StaticDialog() {};
	void init(HINSTANCE hInst, HWND parent){
		Window::init(hInst, parent);
	};

	int doDialog(const TCHAR *fn, bool isRTL = false);

    virtual void destroy() {
    };

	generic_string getFullFilePath() {
		return _fullFilePath;
	};

protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private :
	generic_string _fullFilePath;

};

#endif // PROJECTPANEL_H
