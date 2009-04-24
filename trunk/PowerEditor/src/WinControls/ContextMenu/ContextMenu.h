/*
this file is part of notepad++
Copyright (C)2003 Don HO ( donho@altern.org )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef CONTEXTMENU
#define CONTEXTMENU

#include <vector>
#include <string>

using namespace std;

struct MenuItemUnit {
	unsigned long _cmdID;
	generic_string _itemName;
	MenuItemUnit() : _cmdID(0), _itemName(TEXT("")) {};
	MenuItemUnit(unsigned long cmdID, generic_string itemName) : _cmdID(cmdID), _itemName(itemName) {};
	MenuItemUnit(unsigned long cmdID, const TCHAR *itemName) : _cmdID(cmdID){
		if (!itemName)
			_itemName = TEXT("");
		else
			_itemName = itemName;
	};
};

class ContextMenu {
public:
	ContextMenu() : _hParent(NULL), _hMenu(NULL) {};
	~ContextMenu() {
		if (isCreated())
			::DestroyMenu(_hMenu);
	};
	void create(HWND hParent, const vector<MenuItemUnit> & menuItemArray) { 
		_hParent = hParent;
		_hMenu = ::CreatePopupMenu();
		for (size_t i = 0 ; i < menuItemArray.size() ; i++)
		{
			unsigned int flag = MF_BYPOSITION | ((menuItemArray[i]._cmdID == 0)?MF_SEPARATOR:0);
			::InsertMenu(_hMenu, i, flag, menuItemArray[i]._cmdID, menuItemArray[i]._itemName.c_str());
		}
	};
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

};

#endif //CONTEXTMENU
