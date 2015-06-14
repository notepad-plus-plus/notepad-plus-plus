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


#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include "Common.h"

struct MenuItemUnit {
	unsigned long _cmdID;
	generic_string _itemName;
	generic_string _parentFolderName;
	MenuItemUnit() : _cmdID(0), _itemName(TEXT("")), _parentFolderName(TEXT("")){};
	MenuItemUnit(unsigned long cmdID, generic_string itemName, generic_string parentFolderName=TEXT(""))
		: _cmdID(cmdID), _itemName(itemName), _parentFolderName(parentFolderName){};
	MenuItemUnit(unsigned long cmdID, const TCHAR *itemName, const TCHAR *parentFolderName=NULL);
};

class ContextMenu {
public:
	ContextMenu() : _hParent(NULL), _hMenu(NULL) {};
	~ContextMenu();

	void create(HWND hParent, const std::vector<MenuItemUnit> & menuItemArray, const HMENU mainMenuHandle = NULL);
	bool isCreated() const {return _hMenu != NULL;};
	
	void display(const POINT & p) const {
		::TrackPopupMenu(_hMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hParent, NULL);
	};

	void enableItem(int cmdID, bool doEnable) const {
		int flag = doEnable?MF_ENABLED | MF_BYCOMMAND:MF_DISABLED | MF_GRAYED | MF_BYCOMMAND;
		::EnableMenuItem(_hMenu, cmdID, flag);
	};

	void checkItem(int cmdID, bool doCheck) const {
		::CheckMenuItem(_hMenu, cmdID, MF_BYCOMMAND | (doCheck?MF_CHECKED:MF_UNCHECKED));
	};

	HMENU getMenuHandle() {
		return _hMenu;
	};

private:
	HWND _hParent;
	HMENU _hMenu;
	std::vector<HMENU> _subMenus;

};

#endif //CONTEXTMENU_H
