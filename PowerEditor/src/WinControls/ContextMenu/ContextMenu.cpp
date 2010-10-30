/*
this file is part of notepad++
Copyright (C)2010 Don HO <donho@altern.org>

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

#include "precompiledHeaders.h"
#include "ContextMenu.h"

MenuItemUnit::MenuItemUnit(unsigned long cmdID, const TCHAR *itemName, const TCHAR *parentFolderName) : _cmdID(cmdID)
{
	if (!itemName)
		_itemName = TEXT("");
	else
		_itemName = itemName;

	if (!parentFolderName)
		_parentFolderName = TEXT("");
	else
		_parentFolderName = parentFolderName;
}

ContextMenu::~ContextMenu()
{
	if (isCreated())
	{
		for (size_t i = 0 ; i < _subMenus.size() ; i++)
			::DestroyMenu(_subMenus[i]);
		::DestroyMenu(_hMenu);
	}
}
	
void ContextMenu::create(HWND hParent, const vector<MenuItemUnit> & menuItemArray)
{ 
	_hParent = hParent;
	_hMenu = ::CreatePopupMenu();
	bool lastIsSep = false;
	HMENU hParentFolder = NULL;
	generic_string currentParentFolderStr = TEXT("");
	int j = 0;

	for (size_t i = 0 ; i < menuItemArray.size() ; i++)
	{
		const MenuItemUnit & item = menuItemArray[i];
		if (item._parentFolderName == TEXT(""))
		{
			currentParentFolderStr = TEXT("");
			hParentFolder = NULL;
			j = 0;
		}
		else
		{
			if (item._parentFolderName != currentParentFolderStr)
			{
				currentParentFolderStr = item._parentFolderName;
				hParentFolder = ::CreateMenu();
				j = 0;

				_subMenus.push_back(hParentFolder);
				::InsertMenu(_hMenu, i, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hParentFolder, currentParentFolderStr.c_str());
			}
		}

		unsigned int flag = MF_BYPOSITION | ((item._cmdID == 0)?MF_SEPARATOR:0);
		if (hParentFolder)
		{
			::InsertMenu(hParentFolder, j++, flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if ((i == 0 || i == menuItemArray.size() - 1) && item._cmdID == 0)
		{
			lastIsSep = true;
		}
		else if (item._cmdID != 0)
		{
			::InsertMenu(_hMenu, i, flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if (item._cmdID == 0 && !lastIsSep)
		{
			::InsertMenu(_hMenu, i, flag, item._cmdID, item._itemName.c_str());
			lastIsSep = true;
		}
		else // last item is separator and current item is separator
		{
			lastIsSep = true;
		}
	}
}
	