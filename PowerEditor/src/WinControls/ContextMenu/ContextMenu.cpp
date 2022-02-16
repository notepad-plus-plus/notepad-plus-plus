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


#include "ContextMenu.h"
#include "menuCmdID.h"
#include "Parameters.h"
#include "localization.h"

MenuItemUnit::MenuItemUnit(unsigned long cmdID, const TCHAR *itemName, const TCHAR *parentFolderName) : _cmdID(cmdID)
{
	if (!itemName)
		_itemName.clear();
	else
		_itemName = itemName;

	if (!parentFolderName)
		_parentFolderName.clear();
	else
		_parentFolderName = parentFolderName;
}


ContextMenu::~ContextMenu()
{
	if (isCreated())
	{
		for (size_t i = 0, len = _subMenus.size(); i < len; ++i)
			::DestroyMenu(_subMenus[i]);
		::DestroyMenu(_hMenu);
	}
}

	
void ContextMenu::create(HWND hParent, const std::vector<MenuItemUnit> & menuItemArray, const HMENU mainMenuHandle, bool copyLink)
{ 
	_hParent = hParent;
	_hMenu = ::CreatePopupMenu();
	bool lastIsSep = false;
	HMENU hParentFolder = NULL;
	generic_string currentParentFolderStr;
	int j = 0;
	MENUITEMINFO mii;

	for (size_t i = 0, len = menuItemArray.size(); i < len; ++i)
	{
		const MenuItemUnit & item = menuItemArray[i];
		if (item._parentFolderName.empty())
		{
			currentParentFolderStr.clear();
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
				::InsertMenu(_hMenu, static_cast<UINT>(i), MF_BYPOSITION | MF_POPUP, (UINT_PTR)hParentFolder, currentParentFolderStr.c_str());
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
			::InsertMenu(_hMenu, static_cast<UINT>(i), flag, item._cmdID, item._itemName.c_str());
			lastIsSep = false;
		}
		else if (item._cmdID == 0 && !lastIsSep)
		{
			::InsertMenu(_hMenu, static_cast<int32_t>(i), flag, item._cmdID, item._itemName.c_str());
			lastIsSep = true;
		}
		else // last item is separator and current item is separator
		{
			lastIsSep = true;
		}
		if (mainMenuHandle)
		{
			UINT s = ::GetMenuState(mainMenuHandle, item._cmdID, MF_BYCOMMAND);
			if (s != ((UINT)-1))
			{
				bool isEnabled = (s & (MF_DISABLED | MF_GRAYED)) == 0;
				bool isChecked = (s & (MF_CHECKED)) != 0;
				if (!isEnabled)
					enableItem(item._cmdID, isEnabled);
				if (isChecked)
					checkItem(item._cmdID, isChecked);
			}

			// set up any menu item bitmaps in the context menu, using main menu bitmaps
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_CHECKMARKS;
			mii.fType = MFT_BITMAP;
			GetMenuItemInfo(mainMenuHandle, item._cmdID, FALSE, &mii);
			SetMenuItemInfo(_hMenu, item._cmdID, FALSE, &mii);
		}

		if (copyLink && (item._cmdID == IDM_EDIT_COPY))
		{
			NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
			generic_string localized = nativeLangSpeaker->getNativeLangMenuString(IDM_EDIT_COPY_LINK);
			if (localized.length() == 0)
				localized = L"Copy link";
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
			mii.wID = IDM_EDIT_COPY_LINK;
			mii.dwTypeData = (TCHAR*) localized.c_str();
			mii.fState = MFS_ENABLED;
			int c = GetMenuItemCount(_hMenu);
			SetMenuItemInfo(_hMenu, c - 1, TRUE, & mii);
		}
	}
}
