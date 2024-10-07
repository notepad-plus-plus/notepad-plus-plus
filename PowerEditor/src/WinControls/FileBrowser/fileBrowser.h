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

#define FB_PANELTITLE         L"Folder as Workspace"
#define FB_ADDROOT            L"Add"
#define FB_REMOVEALLROOTS     L"Remove All"
#define FB_REMOVEROOTFOLDER   L"Remove"
#define FB_COPYPATH           L"Copy path"
#define FB_COPYFILENAME       L"Copy file name"
#define FB_FINDINFILES        L"Find in Files..."
#define FB_EXPLORERHERE       L"Explorer here"
#define FB_CMDHERE            L"CMD here"
#define FB_OPENINNPP          L"Open"
#define FB_SHELLEXECUTE       L"Run by system"

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
	FileInfo(const std::wstring & name, FolderInfo *parent) : _name(name), _parent(parent) {};
	std::wstring getName() const { return _name; };
	void setName(const std::wstring& name) { _name = name; };

private:
	std::wstring _name;
	FolderInfo *_parent = nullptr;
};


class FolderInfo final
{
friend class FileBrowser;
friend class FolderUpdater;

public:
	FolderInfo() = delete; // constructor by default is forbidden
	FolderInfo(const std::wstring & name, FolderInfo *parent) : _name(name), _parent(parent) {};
	void setRootPath(const std::wstring& rootPath) { _rootPath = rootPath; };
	std::wstring getRootPath() const { return _rootPath; };
	void setName(const std::wstring& name) { _name = name; };
	std::wstring getName() const { return _name; };
	void addFile(const std::wstring& fn) { _files.push_back(FileInfo(fn, this)); };
	void addSubFolder(FolderInfo subDirectoryStructure) { _subFolders.push_back(subDirectoryStructure); };

	bool addToStructure(std::wstring & fullpath, std::vector<std::wstring> linarPathArray);
	bool removeFromStructure(std::vector<std::wstring> linarPathArray);
	bool renameInStructure(std::vector<std::wstring> linarPathArrayFrom, std::vector<std::wstring> linarPathArrayTo);

private:
	std::vector<FolderInfo> _subFolders;
	std::vector<FileInfo> _files;
	std::wstring _name;
	FolderInfo* _parent = nullptr;
	std::wstring _rootPath; // set only for root folder; empty for normal folder
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

	static void processChange(DWORD dwAction, std::vector<std::wstring> filesToChange, FolderUpdater* thisFolderUpdater);
};

struct SortingData4lParam {
	std::wstring _rootPath; // Only for the root. It should be empty if it's not root
	std::wstring _label;    // TreeView item label
	bool _isFolder = false;   // if it's not a folder, then it's a file

	SortingData4lParam(std::wstring rootPath, std::wstring label, bool isFolder) : _rootPath(rootPath), _label(label), _isFolder(isFolder) {}
};


class FileBrowser : public DockingDlgInterface {
public:
	FileBrowser(): DockingDlgInterface(IDD_FILEBROWSER) {};
	~FileBrowser();

	void setParent(HWND parent2set){
		_hParent = parent2set;
	};

	void setBackgroundColor(COLORREF bgColour) override {
		TreeView_SetBkColor(_treeView.getHSelf(), bgColour);
	};

	void setForegroundColor(COLORREF fgColour) override {
		TreeView_SetTextColor(_treeView.getHSelf(), fgColour);
	};

	std::wstring getNodePath(HTREEITEM node) const;
	std::wstring getNodeName(HTREEITEM node) const;
	void addRootFolder(std::wstring rootFolderPath);

	HTREEITEM getRootFromFullPath(const std::wstring & rootPath) const;
	HTREEITEM findChildNodeFromName(HTREEITEM parent, const std::wstring& label) const;

	HTREEITEM findInTree(const std::wstring& rootPath, HTREEITEM node, std::vector<std::wstring> linarPathArray) const;

	void deleteAllFromTree() {
		popupMenuCmd(IDM_FILEBROWSER_REMOVEALLROOTS);
	};

	bool renameInTree(const std::wstring& rootPath, HTREEITEM node, const std::vector<std::wstring>& linarPathArrayFrom, const std::wstring & renameTo);

	std::vector<std::wstring> getRoots() const;
	std::wstring getSelectedItemPath() const;

	bool selectItemFromPath(const std::wstring& itemPath) const;

protected:
	HWND _hToolbarMenu = nullptr;
	std::vector<HIMAGELIST> _iconListVector;

	TreeView _treeView;
	HIMAGELIST _hImaLst = nullptr;

	HMENU _hGlobalMenu = NULL;
	HMENU _hRootMenu = NULL;
	HMENU _hFolderMenu = NULL;
	HMENU _hFileMenu = NULL;
	std::vector<FolderUpdater *> _folderUpdaters;

	std::wstring _selectedNodeFullPath; // this member is used only for PostMessage call

	std::vector<SortingData4lParam*> sortingDataArray;

	std::wstring _expandAllFolders = L"Unfold all";
	std::wstring _collapseAllFolders = L"Fold all";
	std::wstring _locateCurrentFile = L"Locate current file";

	void initPopupMenus();
	void destroyMenus();

	BrowserNodeType getNodeType(HTREEITEM hItem);
	void popupMenuCmd(int cmdID);

	bool selectCurrentEditingFile() const;

	struct FilesToChange {
		std::wstring _commonPath; // Common path between all the files. _rootPath + _linarWithoutLastPathElement
		std::wstring _rootPath;
		std::vector<std::wstring> _linarWithoutLastPathElement;
		std::vector<std::wstring> _files; // file/folder names
	};

	std::vector<FilesToChange> getFilesFromParam(LPARAM lParam) const;

	bool addToTree(FilesToChange & group, HTREEITEM node);

	bool deleteFromTree(FilesToChange & group);

	std::vector<HTREEITEM> findInTree(FilesToChange & group, HTREEITEM node) const;

	std::vector<HTREEITEM> findChildNodesFromNames(HTREEITEM parent, std::vector<std::wstring> & labels) const;

	void removeNamesAlreadyInNode(HTREEITEM parent, std::vector<std::wstring> & labels) const;

	intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void openSelectFile();
	void getDirectoryStructure(const wchar_t *dir, const std::vector<std::wstring> & patterns, FolderInfo & directoryStructure, bool isRecursive, bool isInHiddenDir); 
	HTREEITEM createFolderItemsFromDirStruct(HTREEITEM hParentItem, const FolderInfo & directoryStructure);
	static int CALLBACK categorySortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};
