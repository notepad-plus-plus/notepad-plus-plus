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
#include "fileBrowser_rc.h"

#define FB_PANELTITLE         TEXT("Folder as Workspace")
#define FB_ADDROOT            TEXT("Add")
#define FB_REMOVEALLROOTS     TEXT("Remove All")
#define FB_REMOVEROOTFOLDER   TEXT("Remove")
#define FB_COPYPATH           TEXT("Copy path")
#define FB_COPYFILENAME       TEXT("Copy file name")
#define FB_FINDINFILES        TEXT("Find in Files...")
#define FB_EXPLORERHERE       TEXT("Explorer here")
#define FB_CMDHERE            TEXT("CMD here")
#define FB_OPENINNPP          TEXT("Open")
#define FB_SHELLEXECUTE       TEXT("Run by system")

#define FOLDERASWORKSPACE_NODE "FolderAsWorkspace"


class TiXmlNode;
class FileBrowser;
class FolderInfo;

class FileInfo final
{
friend class FileBrowser;
friend class FolderInfo;

public:
	FileInfo() = delete; // constructor by default is forbidden
	FileInfo(const generic_string & name, FolderInfo *parent) : _name(name), _parent(parent) {};
	generic_string getName() const { return _name; };
	void setName(generic_string name) { _name = name; };

private:
	FolderInfo *_parent = nullptr;
	generic_string _name;
};


class FolderInfo final
{
friend class FileBrowser;
friend class FolderUpdater;

public:
	FolderInfo() = delete; // constructor by default is forbidden
	FolderInfo(const generic_string & name, FolderInfo *parent) : _name(name), _parent(parent) {};
	void setRootPath(const generic_string& rootPath) { _rootPath = rootPath; };
	generic_string getRootPath() const { return _rootPath; };
	void setName(const generic_string& name) { _name = name; };
	generic_string getName() const { return _name; };
	void addFile(const generic_string& fn) { _files.push_back(FileInfo(fn, this)); };
	void addSubFolder(FolderInfo subDirectoryStructure) { _subFolders.push_back(subDirectoryStructure); };

	bool addToStructure(generic_string & fullpath, std::vector<generic_string> linarPathArray);
	bool removeFromStructure(std::vector<generic_string> linarPathArray);
	bool renameInStructure(std::vector<generic_string> linarPathArrayFrom, std::vector<generic_string> linarPathArrayTo);

private:
	std::vector<FolderInfo> _subFolders;
	std::vector<FileInfo> _files;
	FolderInfo* _parent = nullptr;
	generic_string _name;
	generic_string _rootPath; // set only for root folder; empty for normal folder
};

enum BrowserNodeType {
	browserNodeType_root = 0, browserNodeType_folder = 2, browserNodeType_file = 3
};

class FolderUpdater {
friend class FileBrowser;
public:
	FolderUpdater(const FolderInfo& fi, FileBrowser *pFileBrowser) : _rootFolder(fi), _pFileBrowser(pFileBrowser) {};
	~FolderUpdater() = default;

	void startWatcher();
	void stopWatcher();

private:
	FolderInfo _rootFolder;
	FileBrowser* _pFileBrowser = nullptr;
	HANDLE _watchThreadHandle = nullptr;
	HANDLE _EventHandle = nullptr;
	static DWORD WINAPI watching(void *param);

	static void processChange(DWORD dwAction, std::vector<generic_string> filesToChange, FolderUpdater* thisFolderUpdater);
};

struct SortingData4lParam {
	generic_string _rootPath; // Only for the root. It should be empty if it's not root
	generic_string _label;    // TreeView item label
	bool _isFolder = false;   // if it's not a folder, then it's a file

	SortingData4lParam(generic_string rootPath, generic_string label, bool isFolder) : _rootPath(rootPath), _label(label), _isFolder(isFolder) {}
};


class FileBrowser : public DockingDlgInterface {
public:
	FileBrowser(): DockingDlgInterface(IDD_FILEBROWSER) {};
	~FileBrowser();
	void init(HINSTANCE hInst, HWND hPere) {
		DockingDlgInterface::init(hInst, hPere);
	}

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	virtual void setBackgroundColor(COLORREF bgColour) {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
    };

	virtual void setForegroundColor(COLORREF fgColour) {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
    };

	generic_string getNodePath(HTREEITEM node) const;
	generic_string getNodeName(HTREEITEM node) const;
	void addRootFolder(generic_string rootFolderPath);

	HTREEITEM getRootFromFullPath(const generic_string & rootPath) const;
	HTREEITEM findChildNodeFromName(HTREEITEM parent, const generic_string& label) const;

	HTREEITEM findInTree(const generic_string& rootPath, HTREEITEM node, std::vector<generic_string> linarPathArray) const;

	void deleteAllFromTree() {
		popupMenuCmd(IDM_FILEBROWSER_REMOVEALLROOTS);
	};

	bool renameInTree(const generic_string& rootPath, HTREEITEM node, const std::vector<generic_string>& linarPathArrayFrom, const generic_string & renameTo);

	std::vector<generic_string> getRoots() const;
	generic_string getSelectedItemPath() const;

	bool selectItemFromPath(const generic_string& itemPath) const;

protected:
	HWND _hToolbarMenu = nullptr;

	TreeView _treeView;
	HIMAGELIST _hImaLst = nullptr;

	HMENU _hGlobalMenu = NULL;
	HMENU _hRootMenu = NULL;
	HMENU _hFolderMenu = NULL;
	HMENU _hFileMenu = NULL;
	std::vector<FolderUpdater *> _folderUpdaters;

	generic_string _selectedNodeFullPath; // this member is used only for PostMessage call

	std::vector<SortingData4lParam*> sortingDataArray;

	generic_string _expandAllFolders = TEXT("Expand all folders");
	generic_string _collapseAllFolders = TEXT("Collapse all folders");
	generic_string _locateCurrentFile = TEXT("Locate current file");

	void initPopupMenus();
	void destroyMenus();

	BrowserNodeType getNodeType(HTREEITEM hItem);
	void popupMenuCmd(int cmdID);

	bool selectCurrentEditingFile() const;

	struct FilesToChange {
		generic_string _commonPath; // Common path between all the files. _rootPath + _linarWithoutLastPathElement
		generic_string _rootPath;
		std::vector<generic_string> _linarWithoutLastPathElement;
		std::vector<generic_string> _files; // file/folder names
	};

	std::vector<FilesToChange> getFilesFromParam(LPARAM lParam) const;

	bool addToTree(FilesToChange & group, HTREEITEM node);

	bool deleteFromTree(FilesToChange & group);

	std::vector<HTREEITEM> findInTree(FilesToChange & group, HTREEITEM node) const;

	std::vector<HTREEITEM> findChildNodesFromNames(HTREEITEM parent, std::vector<generic_string> & labels) const;

	void removeNamesAlreadyInNode(HTREEITEM parent, std::vector<generic_string> & labels) const;

	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void openSelectFile();
	void getDirectoryStructure(const TCHAR *dir, const std::vector<generic_string> & patterns, FolderInfo & directoryStructure, bool isRecursive, bool isInHiddenDir); 
	HTREEITEM createFolderItemsFromDirStruct(HTREEITEM hParentItem, const FolderInfo & directoryStructure);
	static int CALLBACK categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};
