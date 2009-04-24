//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
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

#include "lastRecentFileList.h"
#include "menuCmdID.h"


void LastRecentFileList::initMenu(HMENU hMenu, int idBase, int posBase) {
	_hMenu = hMenu;
	_idBase = idBase;
	_posBase = posBase;

	for (int i = 0 ; i < sizeof(_idFreeArray) ; i++)
		_idFreeArray[i] = true;
};

void LastRecentFileList::updateMenu() {
	if (!_hasSeparators && _size > 0) {	//add separators
		const char * nativeLangOpenAllFiles = (NppParameters::getInstance())->getNativeLangMenuStringA(IDM_OPEN_ALL_RECENT_FILE);
		const char * nativeLangCleanFilesList = (NppParameters::getInstance())->getNativeLangMenuStringA(IDM_CLEAN_RECENT_FILE_LIST);

		const char * openAllFileStr = nativeLangOpenAllFiles?nativeLangOpenAllFiles:"Open All Recent Files";
		const char * cleanFileListStr = nativeLangCleanFilesList?nativeLangCleanFilesList:"Clean Recent Files List";
		::InsertMenu(_hMenu, _posBase + 0, MF_BYPOSITION, UINT(-1), 0);
#ifdef UNICODE
		WcharMbcsConvertor *wmc = WcharMbcsConvertor::getInstance();
		const wchar_t * openAllFileStrW = wmc->char2wchar(openAllFileStr, CP_ANSI_LATIN_1);
		::InsertMenu(_hMenu, _posBase + 1, MF_BYPOSITION, IDM_OPEN_ALL_RECENT_FILE, openAllFileStrW);
		const wchar_t * cleanFileListStrW = wmc->char2wchar(cleanFileListStr, CP_ANSI_LATIN_1);
		::InsertMenu(_hMenu, _posBase + 2, MF_BYPOSITION, IDM_CLEAN_RECENT_FILE_LIST, cleanFileListStrW);
#else
		::InsertMenu(_hMenu, _posBase + 1, MF_BYPOSITION, IDM_OPEN_ALL_RECENT_FILE, openAllFileStr);
		::InsertMenu(_hMenu, _posBase + 2, MF_BYPOSITION, IDM_CLEAN_RECENT_FILE_LIST, cleanFileListStr);
#endif
		::InsertMenu(_hMenu, _posBase + 3, MF_BYPOSITION, UINT(-1), 0);
		_hasSeparators = true;

	}
	else if (_hasSeparators && _size == 0) 	//remove separators
	{
		::RemoveMenu(_hMenu, _posBase + 3, MF_BYPOSITION);
		::RemoveMenu(_hMenu, IDM_CLEAN_RECENT_FILE_LIST, MF_BYCOMMAND);
		::RemoveMenu(_hMenu, IDM_OPEN_ALL_RECENT_FILE, MF_BYCOMMAND);
		::RemoveMenu(_hMenu, _posBase + 0, MF_BYPOSITION);
		_hasSeparators = false;

	}

	//Remove all menu items
	for(int i = 0; i < _size; i++) {
		::RemoveMenu(_hMenu, _lrfl.at(i)._id, MF_BYCOMMAND);
	}
	//Then readd them, so everything stays in sync
	TCHAR indexBuffer[4];
	for(int j = 0; j < _size; j++) {
		std::generic_string menuString = TEXT("");
		if (j < 9) {	//first 9 have accelerator (0 unused)
			menuString += TEXT("&");
		}
		wsprintf(indexBuffer, TEXT("%d"), j+1);//one based numbering
		menuString += indexBuffer;	
		menuString += TEXT(" ");
		menuString += _lrfl.at(j)._name;
		::InsertMenu(_hMenu, _posBase + j, MF_BYPOSITION, _lrfl.at(j)._id, menuString.c_str());
	}
}

void LastRecentFileList::add(const TCHAR *fn) {
	if (_userMax == 0 || _locked)
		return;

	RecentItem itemToAdd(fn);

	int index = find(fn);
	if (index != -1) {	//already in list, bump upwards
		remove(index);
	}

	if (_size == _userMax) {
		itemToAdd._id = _lrfl.back()._id;
		_lrfl.pop_back();	//remove oldest
	} else {
		itemToAdd._id = popFirstAvailableID();
		_size++;
	}
	_lrfl.push_front(itemToAdd);
	updateMenu();
};

void LastRecentFileList::remove(const TCHAR *fn) { 
	int index = find(fn);
	if (index != -1)
		remove(index);
};

void LastRecentFileList::remove(int index) {
	if (_size == 0 || _locked)
		return;
	if (index > -1 && index < (int)_lrfl.size()) {
		::RemoveMenu(_hMenu, _lrfl.at(index)._id, MF_BYCOMMAND);
		setAvailable(_lrfl.at(index)._id);
		_lrfl.erase(_lrfl.begin() + index);
		_size--;
		updateMenu();
	}
};


void LastRecentFileList::clear() {
	if (_size == 0)
		return;

	for(int i = (_size-1); i >= 0; i--) {
		::RemoveMenu(_hMenu, _lrfl.at(i)._id, MF_BYCOMMAND);
		setAvailable(_lrfl.at(i)._id);
		_lrfl.erase(_lrfl.begin() + i);
	}
	_size = 0;
	updateMenu();
}


std::generic_string & LastRecentFileList::getItem(int id) {
	int i = 0;
	for(; i < _size; i++) {
		if (_lrfl.at(i)._id == id)
			break;
	}
	if (i == _size)
		i = 0;
	return _lrfl.at(i)._name;	//if not found, return first
};

std::generic_string & LastRecentFileList::getIndex(int index) {
	return _lrfl.at(index)._name;	//if not found, return first
};


void LastRecentFileList::setUserMaxNbLRF(int size) {
	_userMax = size;
	if (_size > _userMax) {	//start popping items
		int toPop = _size-_userMax;
		while(toPop > 0) {
			::RemoveMenu(_hMenu, _lrfl.back()._id, MF_BYCOMMAND);
			setAvailable(_lrfl.back()._id);
			_lrfl.pop_back();
			toPop--;
			_size--;
		}
		updateMenu();
		_size = _userMax;
	}
};



void LastRecentFileList::saveLRFL() {
	NppParameters *pNppParams = NppParameters::getInstance();
	if (pNppParams->writeNbHistoryFile(_userMax))
	{
		for(int i = _size - 1; i >= 0; i--)	//reverse order: so loading goes in correct order
		{
			pNppParams->writeHistory(_lrfl.at(i)._name.c_str());
		}
	}
};



int LastRecentFileList::find(const TCHAR *fn) {
	int i = 0;
	for(int i = 0; i < _size; i++) {
		if (!lstrcmpi(_lrfl.at(i)._name.c_str(), fn)) {
			return i;
		}
	}
	return -1;
};

int LastRecentFileList::popFirstAvailableID() {
	for (int i = 0 ; i < NB_MAX_LRF_FILE ; i++)
	{
		if (_idFreeArray[i])
		{
			_idFreeArray[i] = false;
			return i + _idBase;
		}
	}
	return 0;
};

void LastRecentFileList::setAvailable(int id) {
	int index = id - _idBase;
	_idFreeArray[index] = true;
};
