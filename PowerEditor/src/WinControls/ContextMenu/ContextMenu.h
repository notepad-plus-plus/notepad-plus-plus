// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
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
#pragma once
#include "Common.h"



struct MenuItemUnit final
{
	unsigned long _cmdID = 0;
	generic_string _itemName;
	generic_string _parentFolderName;

	MenuItemUnit() = default;
	MenuItemUnit(unsigned long cmdID, const generic_string& itemName, const generic_string& parentFolderName = generic_string())
		: _cmdID(cmdID), _itemName(itemName), _parentFolderName(parentFolderName){};
	MenuItemUnit(unsigned long cmdID, const TCHAR *itemName, const TCHAR *parentFolderName = nullptr);
};


class ContextMenu final
{
public:
	~ContextMenu();

	void create(HWND hParent, const std::vector<MenuItemUnit> & menuItemArray, const HMENU mainMenuHandle = NULL, bool copyLink = false);
	bool isCreated() const {return _hMenu != NULL;}
	
	void display(const POINT & p) const {
		::TrackPopupMenu(_hMenu, TPM_LEFTALIGN, p.x, p.y, 0, _hParent, NULL);
	}

	void enableItem(int cmdID, bool doEnable) const
	{
		int flag = doEnable ? (MF_ENABLED | MF_BYCOMMAND) : (MF_DISABLED | MF_GRAYED | MF_BYCOMMAND);
		::EnableMenuItem(_hMenu, cmdID, flag);
	}

	void checkItem(int cmdID, bool doCheck) const
	{
		::CheckMenuItem(_hMenu, cmdID, MF_BYCOMMAND | (doCheck ? MF_CHECKED : MF_UNCHECKED));
	}

	HMENU getMenuHandle() const
	{
		return _hMenu;
	}

private:
	HWND _hParent = NULL;
	HMENU _hMenu = NULL;
	std::vector<HMENU> _subMenus;

};
