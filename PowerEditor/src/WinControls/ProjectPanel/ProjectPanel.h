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
#define PM_EDITADDDIRECTORY	       TEXT("Add Directory...")
#define PM_EDITREMOVE              TEXT("Remove\tDEL")
#define PM_EDITMODIFYFILE          TEXT("Modify File Path")
#define PM_SETFILTERS              TEXT("Set Filters...")
#define PM_REFRESH			       TEXT("Refresh")

#define PM_WORKSPACEMENUENTRY      TEXT("Workspace")
#define PM_EDITMENUENTRY           TEXT("Edit")

#define PM_MOVEUPENTRY             TEXT("Move Up\tCtrl+Up")
#define PM_MOVEDOWNENTRY           TEXT("Move Down\tCtrl+Down")

class TiXmlNode;




class ProjectPanel : public DockingDlgInterface, public TreeViewListener {

	friend int CALLBACK compareFunc(LPARAM, LPARAM, LPARAM);

	enum NodeType { nodeType_dummy = -1, nodeType_root = 0, nodeType_project = 1, nodeType_folder = 2, nodeType_file = 3, nodeType_baseDir = 4, nodeType_dir = 5, nodeType_dirFile = 6, };

	// private class ProjectPanelData is stored in the LPARAM of the tree nodes.
	// The data it contains depends on the node type.
	class ProjectPanelData final : public TreeViewData {
	public:
		generic_string _name;
		generic_string _filePath;
		generic_string _userLabel;
		std::vector<generic_string> _filters;
		NodeType _nodeType;
		DirectoryWatcher& _directoryWatcher;
		HTREEITEM _hItem;
		bool _watch;

		ProjectPanelData(DirectoryWatcher& directoryWatcher, const TCHAR* name, const TCHAR* filePath, NodeType nodeType, const std::vector<generic_string> filters = std::vector<generic_string>()) 
			: TreeViewData()
			, _name(name)
			, _nodeType(nodeType)
			, _directoryWatcher(directoryWatcher)
			, _hItem(NULL)
			, _watch(false)
			, _filters(filters)
		{
			if (filePath != NULL)
				_filePath = filePath;
		}

		virtual ~ProjectPanelData() {
			setItem(NULL);
		}

		void setItem(HTREEITEM hItem) {
			if (_hItem && _hItem != hItem && _watch)
				watchDir(false);
			_hItem = hItem;
		}

		bool watchDir(bool watch) {
			if (_hItem && _watch && !watch)
				if (_nodeType == nodeType_baseDir || _nodeType == nodeType_dir)
					_directoryWatcher.removeDir(_hItem);

			_watch = watch;
			if( !watch)
				return true;

			if (_hItem)
				if (watch && (_nodeType == nodeType_baseDir || _nodeType == nodeType_dir))
					_directoryWatcher.addOrChangeDir(_filePath,_hItem,_filters);
				else
					return true;

			return !(_nodeType == nodeType_baseDir || _nodeType == nodeType_dir);
		}

		bool isWatching() const {
			return (_nodeType == nodeType_baseDir || _nodeType == nodeType_dir) && _watch;
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
		bool isDirectoryFile() const {
			return _nodeType == nodeType_dirFile;
		}
		bool isDirectory() const {
			return _nodeType == nodeType_dir;
		}
		bool isBaseDirectory() const {
			return _nodeType == nodeType_baseDir;
		}
		bool isDummy() const {
			return _nodeType == nodeType_dummy;
		}

		virtual TreeViewData* clone() const override {
			ProjectPanelData* result = new ProjectPanelData(_directoryWatcher, _name.c_str(), _filePath.c_str(), _nodeType, _filters);
			result->_userLabel = _userLabel;
			return result;
		}

		ProjectPanelData(const ProjectPanelData&) = delete;
		ProjectPanelData& operator= (const ProjectPanelData&) = delete;
	};


	// private class TreeUpdaterDirectory:
	// constructs a directory by the current tree items, which are children of the supplied tree item.
	// It can then be synchronized with a freshly read Directory by synchronizeTo()
	// and updates the tree children according to this Directory.
	class TreeUpdaterDirectory final : public Directory {
		ProjectPanel* _projectPanel;
		TreeView* _treeView;
		HTREEITEM _hItem;
		std::map<generic_string,HTREEITEM> _dirMap;
		std::map<generic_string,HTREEITEM> _fileMap;
	public:
		TreeUpdaterDirectory( ProjectPanel *projectPanel, HTREEITEM hItem );
		virtual ~TreeUpdaterDirectory() {}

	private:

		virtual void onDirAdded(const generic_string& name) override;
		virtual void onDirRemoved(const generic_string& name) override;
		virtual void onFileAdded(const generic_string& name) override;
		virtual void onFileRemoved(const generic_string& name) override;
		virtual void onEndSynchronize(const Directory& other) override;

	};
	
	generic_string _infotipStr;

public:
	ProjectPanel()
		: DockingDlgInterface(IDD_PROJECTPANEL)
		, _treeView()
		, _hToolbarMenu(NULL)
		, _hWorkSpaceMenu(NULL)
		, _hProjectMenu(NULL)
		, _hFolderMenu(NULL)
		, _hFileMenu(NULL)
		, _hDirectoryMenu(NULL)
	{
	};

	virtual ~ProjectPanel() {
	}


	void init(HINSTANCE hInst, HWND hPere) {
		DockingDlgInterface::init(hInst, hPere);
	}

    virtual void display(bool toShow = true) const override {
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

	virtual void setBackgroundColor(COLORREF bgColour) override {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
    };
	virtual void setForegroundColor(COLORREF fgColour) override {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
    };

	TreeView& getTreeView() { return _treeView; }

protected:
	TreeView _treeView;
	HIMAGELIST _hImaLst;
	HWND _hToolbarMenu;
	HMENU _hWorkSpaceMenu, _hProjectMenu, _hFolderMenu, _hFileMenu, _hDirectoryMenu;
	generic_string _workSpaceFilePath;
	generic_string _selDirOfFilesFromDirDlg;
	bool _isDirty;
	DirectoryWatcher _directoryWatcher;

	void initMenus();
	void destroyMenus();
	BOOL setImageList( int root_clean_id, 
	                   int root_dirty_id, 
					   int project_id, 
					   int open_node_id, 
					   int closed_node_id, 
					   int leaf_id, 
					   int ivalid_leaf_id, 
					   int open_dir_id, 
					   int closed_dir_id, 
					   int open_basedir_id, 
					   int closed_basedir_id, 
					   int invalid_basedir_id, 
					   int leaf_dirfile_id
					   );
	void addFiles(HTREEITEM hTreeItem);
	void addFilesFromDirectory(HTREEITEM hTreeItem);
	HTREEITEM addDirectory(HTREEITEM hTreeItem);
	void recursiveAddFilesFrom(const TCHAR *folderPath, HTREEITEM hTreeItem);
	HTREEITEM addFolder(HTREEITEM hTreeItem, const TCHAR *folderName);
	HTREEITEM addDirectory(HTREEITEM hTreeItem, const TCHAR *folderName, bool root = false, const TCHAR *monitorPath = NULL, const std::vector<generic_string>* filters = NULL);

	bool writeWorkSpace(TCHAR *projectFileName = NULL);
	generic_string getRelativePath(const generic_string & fn, const TCHAR *workSpaceFileName);
	void buildProjectXml(TiXmlNode *root, HTREEITEM hItem, const TCHAR* fn2write);
	NodeType getNodeType(HTREEITEM hItem) const;
	void setWorkSpaceDirty(bool isDirty);
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplyPoint(int iButton);
	const std::vector<generic_string>* getFilters(HTREEITEM hItem);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	bool buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem);
	void rebuildDirectoryTree(HTREEITEM hParentItem, const ProjectPanelData& data);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	generic_string getAbsoluteFilePath(const TCHAR * relativePath);
	void openSelectFile();
	HMENU getContextMenu(HTREEITEM hTreeItem) const;
	void expandOrCollapseDirectory(bool expand, HTREEITEM hItem);

	bool setFilters(const std::vector<generic_string>& filters, HTREEITEM hItem);
	void updateBaseDirName(HTREEITEM hItem);

	void removeDummies(HTREEITEM hTreeItem);

	void treeItemChanged(HTREEITEM hItem,TreeViewData* tvdata);

	static generic_string buildDirectoryName(const generic_string& filePath, const std::vector<generic_string>& filters = std::vector<generic_string>());

	void itemVisibilityChanges(HTREEITEM hItem, bool visible);

	// TreeViewListener
	virtual void onTreeItemAdded(bool afterClone, HTREEITEM hItem, TreeViewData* newData) override;
	virtual void onMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) override;
	//

};

class FileRelocalizerDlg final : public StaticDialog
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

class FilterDlg final: public StaticDialog
{
	std::vector<generic_string> _filters;

public :
	FilterDlg() : StaticDialog() {};
	void init(HINSTANCE hInst, HWND parent){
		Window::init(hInst, parent);
	};

	int doDialog(bool isRTL = false);

	const std::vector<generic_string>& getFilters() const { 
		return _filters; 
	}


    virtual void destroy() {
    };


protected :
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

	void addText2Combo(const TCHAR * txt2add, HWND hCombo);
	generic_string getTextFromCombo(HWND hCombo, bool) const;
	void fillComboHistory(int id, const std::vector<generic_string> & strings);
	int saveComboHistory(int id, int maxcount, std::vector<generic_string> & strings);

};

#endif // PROJECTPANEL_H
