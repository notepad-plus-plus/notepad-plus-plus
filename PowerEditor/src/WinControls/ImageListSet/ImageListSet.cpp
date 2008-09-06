//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "ImageListSet.h"
//#include "resource.h"

void ToolBarIcons::init(ToolBarButtonUnit *buttonUnitArray, int arraySize)
{
	for (int i = 0 ; i < arraySize ; i++)
		_tbiis.push_back(buttonUnitArray[i]);
	_nbCmd = arraySize;
}

void ToolBarIcons::create(HINSTANCE hInst, int iconSize)
{
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	//_iconListVector.push_back(IconList());

	_iconListVector[HLIST_DEFAULT].create(hInst, iconSize);
	_iconListVector[HLIST_HOT].create(hInst, iconSize);
	_iconListVector[HLIST_DISABLE].create(hInst, iconSize);
	//_iconListVector[HLIST_UGLY].create(hInst, 16);

	reInit(iconSize);
}

void ToolBarIcons::destroy()
{
	_iconListVector[HLIST_DEFAULT].destroy();
	_iconListVector[HLIST_HOT].destroy();
	_iconListVector[HLIST_DISABLE].destroy();
	//_iconListVector[HLIST_UGLY].destroy();
}
/*
bool IconList::changeIcon(int index, const TCHAR *iconLocation) const 
{
	HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, iconLocation, IMAGE_ICON, _iconSize, _iconSize, LR_LOADFROMFILE	);
	if (!hBmp)
		return false;
	int i = ImageList_ReplaceIcon(_hImglst, index, (HICON)hBmp);
	::DeleteObject(hBmp);
	return (i == index);
}
*/


