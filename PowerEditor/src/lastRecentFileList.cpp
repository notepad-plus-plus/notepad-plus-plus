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



#include "lastRecentFileList.h"
#include "menuCmdID.h"
#include "localization.h"

using namespace std;

void LastRecentFileList::initMenu(HMENU hMenu, int idBase, int posBase, Accelerator *pAccelerator, bool doSubMenu)
{
	if (doSubMenu)
	{
		_hParentMenu = hMenu;
		_hMenu = ::CreatePopupMenu();
	}
	else
	{
		_hParentMenu = NULL;
		_hMenu = hMenu;
	}

	_idBase = idBase;
	_posBase = posBase;
	_pAccelerator = pAccelerator;
	_nativeLangEncoding = NPP_CP_WIN_1252;

	for (size_t i = 0 ; i < sizeof(_idFreeArray) ; ++i)
		_idFreeArray[i] = true;
}


void LastRecentFileList::switchMode()
{
	//Remove all menu items
	::RemoveMenu(_hMenu, IDM_FILE_RESTORELASTCLOSEDFILE, MF_BYCOMMAND);
	::RemoveMenu(_hMenu, IDM_OPEN_ALL_RECENT_FILE, MF_BYCOMMAND);
	::RemoveMenu(_hMenu, IDM_CLEAN_RECENT_FILE_LIST, MF_BYCOMMAND);

	for (int i = 0; i < _size; ++i)
	{
		::RemoveMenu(_hMenu, _lrfl.at(i)._id, MF_BYCOMMAND);
	}

	if (_hParentMenu == NULL) // mode main menu
	{	if (_size > 0)
		{
			::RemoveMenu(_hMenu, _posBase, MF_BYPOSITION);
			::RemoveMenu(_hMenu, _posBase, MF_BYPOSITION);
		}
		// switch to sub-menu mode
		_hParentMenu = _hMenu;
		_hMenu = ::CreatePopupMenu();
		::RemoveMenu(_hMenu, _posBase+1, MF_BYPOSITION);
	}
	else // mode sub-menu
	{
		if (_size > 0)
		{
			::RemoveMenu(_hParentMenu, _posBase, MF_BYPOSITION);
			::RemoveMenu(_hParentMenu, _posBase, MF_BYPOSITION);
		}
		// switch to main menu mode
		::DestroyMenu(_hMenu);
		_hMenu = _hParentMenu;
		_hParentMenu = NULL;
	}
	_hasSeparators = false;
}

void LastRecentFileList::updateMenu()
{
	NppParameters& nppParam = NppParameters::getInstance();

	if (!_hasSeparators && _size > 0) 
	{	
		//add separators
		NativeLangSpeaker *pNativeLangSpeaker = nppParam.getNativeLangSpeaker();

		wstring recentFileList = pNativeLangSpeaker->getSubMenuEntryName("file-recentFiles");
		wstring openRecentClosedFile = pNativeLangSpeaker->getNativeLangMenuString(IDM_FILE_RESTORELASTCLOSEDFILE);
		wstring openAllFiles = pNativeLangSpeaker->getNativeLangMenuString(IDM_OPEN_ALL_RECENT_FILE);
		wstring cleanFileList = pNativeLangSpeaker->getNativeLangMenuString(IDM_CLEAN_RECENT_FILE_LIST);

		if (recentFileList == L"")
			recentFileList = L"&Recent Files";
		if (openRecentClosedFile == L"")
			openRecentClosedFile = L"Restore Recent Closed File";
		if (openAllFiles == L"")
			openAllFiles = L"Open All Recent Files";
		if (cleanFileList == L"")
			cleanFileList = L"Empty Recent Files List";

		if (!isSubMenuMode())
			::InsertMenu(_hMenu, _posBase + 0, MF_BYPOSITION, static_cast<UINT_PTR>(-1), 0);

		::InsertMenu(_hMenu, _posBase + 1, MF_BYPOSITION, IDM_FILE_RESTORELASTCLOSEDFILE, openRecentClosedFile.c_str());
		::InsertMenu(_hMenu, _posBase + 2, MF_BYPOSITION, IDM_OPEN_ALL_RECENT_FILE, openAllFiles.c_str());
		::InsertMenu(_hMenu, _posBase + 3, MF_BYPOSITION, IDM_CLEAN_RECENT_FILE_LIST, cleanFileList.c_str());
		::InsertMenu(_hMenu, _posBase + 4, MF_BYPOSITION, static_cast<UINT_PTR>(-1), 0);
		_hasSeparators = true;

		if (isSubMenuMode())
		{
			::InsertMenu(_hParentMenu, _posBase + 0, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(_hMenu), (LPCTSTR)recentFileList.c_str());
			::InsertMenu(_hParentMenu, _posBase + 1, MF_BYPOSITION, static_cast<UINT_PTR>(-1), 0);
		}
	}
	else if (_hasSeparators && _size == 0) 	//remove separators
	{
		::RemoveMenu(_hMenu, _posBase + 4, MF_BYPOSITION);
		::RemoveMenu(_hMenu, IDM_CLEAN_RECENT_FILE_LIST, MF_BYCOMMAND);
		::RemoveMenu(_hMenu, IDM_OPEN_ALL_RECENT_FILE, MF_BYCOMMAND);
		::RemoveMenu(_hMenu, IDM_FILE_RESTORELASTCLOSEDFILE, MF_BYCOMMAND);
		::RemoveMenu(_hMenu, _posBase + 0, MF_BYPOSITION);
		_hasSeparators = false;

		if (isSubMenuMode())
		{
			// Remove "Recent Files" Entry and the separator from the main menu
			::RemoveMenu(_hParentMenu, _posBase + 1, MF_BYPOSITION);
			::RemoveMenu(_hParentMenu, _posBase + 0, MF_BYPOSITION);

			// Remove the last left separator from the submenu
			::RemoveMenu(_hMenu, 0, MF_BYPOSITION);
		}
	}

	_pAccelerator->updateFullMenu();

	//Remove all menu items
	for (int i = 0; i < _size; ++i) 
	{
		::RemoveMenu(_hMenu, _lrfl.at(i)._id, MF_BYCOMMAND);
	}
	//Then readd them, so everything stays in sync
	for (int j = 0; j < _size; ++j)
	{
		wstring strBuffer(BuildMenuFileName(nppParam.getRecentFileCustomLength(), j, _lrfl.at(j)._name));
		::InsertMenu(_hMenu, _posBase + j, MF_BYPOSITION, _lrfl.at(j)._id, strBuffer.c_str());
	}
	
}

void LastRecentFileList::add(const wchar_t *fn) 
{
	if (_userMax == 0 || _locked)
		return;

	RecentItem itemToAdd(fn);

	int index = find(fn);
	if (index != -1)
	{
		//already in list, bump upwards
		remove(index);
	}

	if (_size == _userMax)
	{
		itemToAdd._id = _lrfl.back()._id;
		_lrfl.pop_back();	//remove oldest
	}
	else
	{
		itemToAdd._id = popFirstAvailableID();
		++_size;
	}
	_lrfl.push_front(itemToAdd);
	updateMenu();
}

void LastRecentFileList::remove(const wchar_t *fn) 
{ 
	int index = find(fn);
	if (index != -1)
		remove(index);
}

void LastRecentFileList::remove(size_t index) 
{
	if (_size == 0 || _locked)
		return;
	if (index < _lrfl.size())
	{
		::RemoveMenu(_hMenu, _lrfl.at(index)._id, MF_BYCOMMAND);
		setAvailable(_lrfl.at(index)._id);
		_lrfl.erase(_lrfl.begin() + index);
		--_size;
		updateMenu();
	}
}


void LastRecentFileList::clear() 
{
	if (_size == 0)
		return;

	for (int i = (_size-1); i >= 0; i--) 
	{
		::RemoveMenu(_hMenu, _lrfl.at(i)._id, MF_BYCOMMAND);
		setAvailable(_lrfl.at(i)._id);
		_lrfl.erase(_lrfl.begin() + i);
	}
	_size = 0;
	updateMenu();
}


wstring & LastRecentFileList::getItem(int id) 
{
	int i = 0;
	for (; i < _size; ++i)
	{
		if (_lrfl.at(i)._id == id)
			break;
	}

	if (i == _size)
		i = 0;

	return _lrfl.at(i)._name;	//if not found, return first
}

wstring & LastRecentFileList::getIndex(int index)
{
	return _lrfl.at(index)._name;	//if not found, return first
}


void LastRecentFileList::setUserMaxNbLRF(int size)
{
	_userMax = size;
	if (_size > _userMax) 
	{	//start popping items
		int toPop = _size-_userMax;
		while (toPop > 0) 
		{
			::RemoveMenu(_hMenu, _lrfl.back()._id, MF_BYCOMMAND);
			setAvailable(_lrfl.back()._id);
			_lrfl.pop_back();
			toPop--;
			_size--;
		}

		updateMenu();
		_size = _userMax;
	}
}



void LastRecentFileList::saveLRFL()
{
	NppParameters& nppParams = NppParameters::getInstance();
	if (nppParams.writeRecentFileHistorySettings(_userMax))
	{
		for (int i = _size - 1; i >= 0; i--)	//reverse order: so loading goes in correct order
		{
			nppParams.writeHistory(_lrfl.at(i)._name.c_str());
		}
	}
}


int LastRecentFileList::find(const wchar_t *fn)
{
	for (int i = 0; i < _size; ++i)
	{
		if (wcscmp(_lrfl.at(i)._name.c_str(), fn) == 0)
		{
			return i;
		}
	}
	return -1;
}

int LastRecentFileList::popFirstAvailableID() 
{
	for (int i = 0 ; i < NB_MAX_LRF_FILE ; ++i)
	{
		if (_idFreeArray[i])
		{
			_idFreeArray[i] = false;
			return i + _idBase;
		}
	}
	return 0;
}

void LastRecentFileList::setAvailable(int id)
{
	int index = id - _idBase;
	_idFreeArray[index] = true;
}
