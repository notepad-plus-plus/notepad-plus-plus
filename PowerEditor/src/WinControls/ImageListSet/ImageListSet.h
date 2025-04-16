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
	void init(HINSTANCE hInst, int iconSize);
	void create(int iconSize, HINSTANCE hInst, int *iconIDArray, int iconIDArraySize);

	void destroy() {
		ImageList_Destroy(_hImglst);
	};

	void removeAll() {
		ImageList_RemoveAll(_hImglst);
	};

	HIMAGELIST getHandle() const {return _hImglst;};
	void addIcon(int iconID, int cx = 16, int cy = 16, int failIconID = -1, bool isToolbarNormal = false) const;
	void addIcon(HICON hIcon) const;

	bool changeIcon(size_t index, const wchar_t *iconLocation) const;

private :
	HIMAGELIST _hImglst = nullptr;
	HINSTANCE _hInst = nullptr;
	int *_pIconIDArray = nullptr;
	int _iconIDArraySize = 0;
	int _iconSize = 0;

	bool changeFluentIconColor(HICON* phIcon, const std::vector<std::pair<COLORREF, COLORREF>>& colorMappings, int tolerance = 3) const;
	bool changeFluentIconColor(HICON* phIcon) const;
};

struct ToolBarButtonUnit
{	
	int _cmdID;

	int _defaultIcon;
	int _grayIcon;

	int _defaultIcon2;
	int _grayIcon2;

	int _defaultDarkModeIcon;
	int _grayDarkModeIcon;

	int _defaultDarkModeIcon2;
	int _grayDarkModeIcon2;

	int _stdIcon;
};

struct DynamicCmdIcoBmp {
	UINT _message = 0;         // identification of icon in tool bar (menu ID)
	HBITMAP _hBmp = nullptr;   // bitmap for toolbar
	HICON _hIcon = nullptr;    // icon for toolbar
	HICON _hIcon_DM = nullptr; // dark mode icon for toolbar
};

typedef std::vector<ToolBarButtonUnit> ToolBarIconIDs;

// Light Mode list
const int HLIST_DEFAULT = 0;
const int HLIST_DISABLE = 1;
const int HLIST_DEFAULT2 = 2;
const int HLIST_DISABLE2 = 3;
// Dark Mode list
const int HLIST_DEFAULT_DM = 4;
const int HLIST_DISABLE_DM = 5;
const int HLIST_DEFAULT_DM2 = 6;
const int HLIST_DISABLE_DM2 = 7;

class ToolBarIcons
{
public :
	ToolBarIcons() = default;

	void init(ToolBarButtonUnit *buttonUnitArray, int arraySize, const std::vector<DynamicCmdIcoBmp>& cmds2add);
	void create(HINSTANCE hInst, int iconSize);
	void destroy();

	HIMAGELIST getDefaultLst() const {
		return _iconListVector[HLIST_DEFAULT].getHandle();
	};

	HIMAGELIST getDisableLst() const {
		return _iconListVector[HLIST_DISABLE].getHandle();
	};

	HIMAGELIST getDefaultLstSet2() const {
		return _iconListVector[HLIST_DEFAULT2].getHandle();
	};

	HIMAGELIST getDisableLstSet2() const {
		return _iconListVector[HLIST_DISABLE2].getHandle();
	};

	HIMAGELIST getDefaultLstDM() const {
		return _iconListVector[HLIST_DEFAULT_DM].getHandle();
	};

	HIMAGELIST getDisableLstDM() const {
		return _iconListVector[HLIST_DISABLE_DM].getHandle();
	};

	HIMAGELIST getDefaultLstSetDM2() const {
		return _iconListVector[HLIST_DEFAULT_DM2].getHandle();
	};

	HIMAGELIST getDisableLstSetDM2() const {
		return _iconListVector[HLIST_DISABLE_DM2].getHandle();
	};

	void resizeIcon(int size) {
		reInit(size);
	};

	void reInit(int size);

	int getStdIconAt(int i) const {
		return _tbiis[i]._stdIcon;
	};

	bool replaceIcon(size_t witchList, size_t iconIndex, const wchar_t *iconLocation) const {
		if ((witchList != HLIST_DEFAULT) && (witchList != HLIST_DISABLE) && 
			(witchList != HLIST_DEFAULT2) && (witchList != HLIST_DISABLE2) &&
			(witchList != HLIST_DEFAULT_DM) && (witchList != HLIST_DISABLE_DM) && 
			(witchList != HLIST_DEFAULT_DM2) && (witchList != HLIST_DISABLE_DM2))
			return false;

		return _iconListVector[witchList].changeIcon(iconIndex, iconLocation);
	};

private :
	ToolBarIconIDs _tbiis;
	std::vector<DynamicCmdIcoBmp> _moreCmds;

	std::vector<IconList> _iconListVector;
};
