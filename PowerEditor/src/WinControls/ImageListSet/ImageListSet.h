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

#include <vector>
#include <windows.h>
#include <commctrl.h>

#define	IDI_SEPARATOR_ICON -1

class IconList
{
public :
	IconList() = default;
	void create(HINSTANCE hInst, int iconSize);
	void create(int iconSize, HINSTANCE hInst, int *iconIDArray, int iconIDArraySize);

	void destroy() {
		ImageList_Destroy(_hImglst);
	};
	HIMAGELIST getHandle() const {return _hImglst;};
	void addIcon(int iconID) const;
	bool changeIcon(int index, const TCHAR *iconLocation) const;
	void setIconSize(int size) const;

private :
	HIMAGELIST _hImglst = nullptr;
	HINSTANCE _hInst = nullptr;
	int *_pIconIDArray = nullptr;
	int _iconIDArraySize = 0;
	int _iconSize = 0;
};

typedef struct 
{	
	int _cmdID;

	int _defaultIcon;
	int _grayIcon;

	int _stdIcon;
} ToolBarButtonUnit;

typedef std::vector<ToolBarButtonUnit> ToolBarIconIDs;

typedef std::vector<IconList> IconListVector;

class IconLists
{
public :
	IconLists() = default;
	HIMAGELIST getImageListHandle(int index) const {
		return _iconListVector[index].getHandle();
	};

protected :
	IconListVector _iconListVector;
};

const int HLIST_DEFAULT = 0;
const int HLIST_DISABLE = 1;

class ToolBarIcons : public IconLists
{
public :
	ToolBarIcons() = default;

	void init(ToolBarButtonUnit *buttonUnitArray, int arraySize);
	void create(HINSTANCE hInst, int iconSize);
	void destroy();

	HIMAGELIST getDefaultLst() const {
		return IconLists::getImageListHandle(HLIST_DEFAULT);
	};

	HIMAGELIST getDisableLst() const {
		return IconLists::getImageListHandle(HLIST_DISABLE);
	};

	void resizeIcon(int size) {
		reInit(size);
	};

	void reInit(int size);

	int getStdIconAt(int i) const {
		return _tbiis[i]._stdIcon;
	};

	bool replaceIcon(int witchList, int iconIndex, const TCHAR *iconLocation) const {
		if ((witchList != HLIST_DEFAULT) /*&& (witchList != HLIST_HOT)*/ && (witchList != HLIST_DISABLE))
			return false;
		return _iconListVector[witchList].changeIcon(iconIndex, iconLocation);
	};

	bool addIcon(HICON hIcon) {
		HIMAGELIST l0 = getDefaultLst();
		HIMAGELIST l2 = getDisableLst();

		int res = ImageList_ReplaceIcon(l0, -1, hIcon);
		if (res == -1) return false;

		res = ImageList_ReplaceIcon(l2, -1, hIcon);
		if (res == -1) return false;

		return true;
	}

private :
	ToolBarIconIDs _tbiis;
};

