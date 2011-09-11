/*
this file is part of notepad++
Copyright (C)2011 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a Copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef PROJECTPANEL_H
#define  PROJECTPANEL_H

//#include <windows.h>
#ifndef DOCKINGDLGINTERFACE_H
#include "DockingDlgInterface.h"
#endif //DOCKINGDLGINTERFACE_H

#include "TreeView.h"
#include "ProjectPanel_rc.h"

#define IDM_PROJECT_RENAME       2560
#define IDM_PROJECT_NEWFOLDER    2561
#define IDM_PROJECT_ADDFILES     2562
#define IDM_PROJECT_DELETEFOLDER 2563
#define IDM_PROJECT_DELETEFILE   2564


class TiXmlNode;

class ProjectPanel : public DockingDlgInterface {
public:
	ProjectPanel(): DockingDlgInterface(IDD_PROJECTPANEL),\
		_hRootMenu(NULL), _hFolderMenu(NULL), _hFileMenu(NULL){};

	void init(HINSTANCE hInst, HWND hPere) {
		DockingDlgInterface::init(hInst, hPere);

		_hRootMenu = ::CreatePopupMenu();
		::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
		::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, TEXT("New Folder..."));
		::InsertMenu(_hRootMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, TEXT("Add Files..."));

		_hFolderMenu = ::CreatePopupMenu();
		::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
		::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_NEWFOLDER, TEXT("New Folder..."));
		::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFOLDER, TEXT("Delete"));
		::InsertMenu(_hFolderMenu, 0, MF_BYCOMMAND, IDM_PROJECT_ADDFILES, TEXT("Add Files..."));

		_hFileMenu = ::CreatePopupMenu();
		::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_RENAME, TEXT("Rename"));
		::InsertMenu(_hFileMenu, 0, MF_BYCOMMAND, IDM_PROJECT_DELETEFILE, TEXT("Delete"));
	};

	void destroy() {
		::DestroyMenu(_hRootMenu);
		::DestroyMenu(_hFolderMenu);
		::DestroyMenu(_hFileMenu);
    };

    virtual void display(bool toShow = true) const {
        DockingDlgInterface::display(toShow);
    };

    void setParent(HWND parent2set){
        _hParent = parent2set;
    };

	bool openProject(TCHAR *projectFileName);
	
protected:
	TreeView _treeView;
	HMENU _hRootMenu, _hFolderMenu, _hFileMenu;
	void popupMenuCmd(int cmdID);
	virtual BOOL CALLBACK ProjectPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);

};
#endif // PROJECTPANEL_H
