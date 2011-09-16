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

enum NodeType {
	nodeType_root = 0, nodeType_node = 1, nodeType_leaf = 2
};

class TiXmlNode;

class ProjectPanel : public DockingDlgInterface {
public:
	ProjectPanel(): DockingDlgInterface(IDD_PROJECTPANEL),\
		_hToolbarMenu(NULL), _hProjectMenu(NULL), _hRootMenu(NULL), _hFolderMenu(NULL), _hFileMenu(NULL){};

	void init(HINSTANCE hInst, HWND hPere);

	void destroy() {
		::DestroyMenu(_hProjectMenu);
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
	void addFiles(HTREEITEM hTreeItem);
	
protected:
	TreeView _treeView;
	HWND _hToolbarMenu;
	HMENU _hProjectMenu, _hRootMenu, _hFolderMenu, _hFileMenu;
	void popupMenuCmd(int cmdID);
	POINT getMenuDisplyPoint(int iButton);
	virtual BOOL CALLBACK ProjectPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	bool buildTreeFrom(TiXmlNode *projectRoot, HTREEITEM hParentItem);
	void notified(LPNMHDR notification);
	void showContextMenu(int x, int y);
	NodeType getNodeType(HTREEITEM hItem);

};
#endif // PROJECTPANEL_H
