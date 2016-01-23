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


#ifndef FILEBROWSER_H
#define  FILEBROWSER_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "TreeView.h"
#include "fileBrowser_rc.h"

#define PM_NEWFOLDERNAME         TEXT("Folder Name")
#define PM_NEWPROJECTNAME        TEXT("Project Name")

#define PM_SAVEWORKSPACE           TEXT("Save")
#define PM_SAVEASWORKSPACE         TEXT("Save As...")
#define PM_SAVEACOPYASWORKSPACE    TEXT("Save a Copy As...")
#define PM_NEWPROJECTWORKSPACE     TEXT("Add New Project")

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

class TiXmlNode;

class changeInfo final
{
friend class FolderInfo;
public:
	enum folderChangeAction{
		add, remove, rename
	};

private:
	bool isFile; // true: file, false: folder
	generic_string _fullFilePath;
	std::vector<generic_string> _relativePath;
	folderChangeAction _action;
};

class FileInfo final
{
friend class FileBrowser;
friend class FolderInfo;

public:
	FileInfo(const generic_string & fn) { _path = fn; };
	generic_string getLabel() { return ::PathFindFileName(_path.c_str()); };

private:
	generic_string _path;
};

class FolderInfo final
{
friend class FileBrowser;
friend class FolderUpdater;

public:
	void setPath(generic_string dn) { _path = dn; };
	void addFile(generic_string fn) { _files.push_back(FileInfo(fn)); };
	void addSubFolder(FolderInfo subDirectoryStructure) { _subFolders.push_back(subDirectoryStructure); };
	bool compare(const FolderInfo & struct2compare, std::vector<changeInfo> & result);
	static bool makeDiff(FolderInfo & struct1, FolderInfo & struct2static, std::vector<changeInfo> result);
	generic_string getLabel();

private:
	std::vector<FolderInfo> _subFolders;
	std::vector<FileInfo> _files;
	generic_string _path;
	generic_string _contentHash;
};

enum BrowserNodeType {
	browserNodeType_root = 0, browserNodeType_folder = 2, browserNodeType_file = 3
};

class FolderUpdater {
friend class FileBrowser;
public:
	FolderUpdater(FolderInfo fi, HWND hFileBrowser) : _rootFolder(fi), _hFileBrowser(hFileBrowser) {};
	~FolderUpdater() {};
	bool updateTree(changeInfo changeInfo); // postMessage to FileBrowser to upgrade GUI

	void startWatcher();
	void stopWatcher();


private:
	FolderInfo _rootFolder;
	HWND _hFileBrowser = nullptr;
	bool _toBeContinued = true;
	HANDLE _watchThreadHandle = nullptr;
	HANDLE _mutex = nullptr;

	static DWORD WINAPI watching(void *param);
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

	void addRootFolder(generic_string);

protected:
	TreeView _treeView;
	HIMAGELIST _hImaLst;
	HWND _hToolbarMenu = NULL;
	HMENU _hWorkSpaceMenu = NULL;
	HMENU _hProjectMenu = NULL;
	HMENU _hFolderMenu = NULL;
	HMENU _hFileMenu = NULL;
	std::vector<FolderUpdater> _folderUpdaters;

	void initMenus();
	void destroyMenus();
	BOOL setImageList(int root_clean_id, int root_dirty_id, int project_id, int open_node_id, int closed_node_id, int leaf_id, int ivalid_leaf_id);
	void addFiles(HTREEITEM hTreeItem);
	void recursiveAddFilesFrom(const TCHAR *folderPath, HTREEITEM hTreeItem);
	HTREEITEM addFolder(HTREEITEM hTreeItem, const TCHAR *folderName);

	generic_string getRelativePath(const generic_string & fn, const TCHAR *workSpaceFileName);
	void buildProjectXml(TiXmlNode *root, HTREEITEM hItem, const TCHAR* fn2write);
	BrowserNodeType getNodeType(HTREEITEM hItem);
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplayPoint(int iButton);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	void openSelectFile();
	void getDirectoryStructure(const TCHAR *dir, const std::vector<generic_string> & patterns, FolderInfo & directoryStructure, bool isRecursive, bool isInHiddenDir); 
	HTREEITEM createFolderItemsFromDirStruct(HTREEITEM hParentItem, const FolderInfo & directoryStructure);
};

#endif // FILEBROWSER_H
