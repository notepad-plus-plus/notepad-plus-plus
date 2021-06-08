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

#include "Parameters.h"
#include <deque>

struct RecentItem {
	int _id = 0;
	generic_string _name;
	explicit RecentItem(const TCHAR * name) : _name(name) {};
};

typedef std::deque<RecentItem> recentList;

class LastRecentFileList
{
public:
	LastRecentFileList() {
		_userMax = (NppParameters::getInstance()).getNbMaxRecentFile();
	};

	void initMenu(HMENU hMenu, int idBase, int posBase, Accelerator *accelerator, bool doSubMenu = false);
	void switchMode();
	void updateMenu();

	void add(const TCHAR *fn);
	void remove(const TCHAR *fn);
	void remove(size_t index);
	void clear();

	int getSize() {
		return _size;
	};


	int getMaxNbLRF() const {
		return NB_MAX_LRF_FILE;
	};

	int getUserMaxNbLRF() const {
		return _userMax;
	};

	generic_string & getItem(int id);	//use menu id
	generic_string & getIndex(int index);	//use menu id

	generic_string getFirstItem() const {
		if (_lrfl.size() == 0)
			return TEXT("");
		return _lrfl.front()._name;
	};

	void setUserMaxNbLRF(int size);

	void saveLRFL();

	void setLock(bool lock) {
		_locked = lock;
	};

	void setLangEncoding(int nativeLangEncoding) {
		_nativeLangEncoding = nativeLangEncoding;
	};

	bool isSubMenuMode() const {
		return (_hParentMenu != NULL);
	};

private:
	recentList _lrfl;
	Accelerator *_pAccelerator = nullptr;
	int _userMax = 0;
	int _size = 0;
	int _nativeLangEncoding = -1;

	// For the menu
	HMENU _hParentMenu = nullptr;
	HMENU _hMenu = nullptr;
	int _posBase = -1;
	int _idBase = -1;
	bool _idFreeArray[NB_MAX_LRF_FILE] = {false};
	bool _hasSeparators = false;
	bool _locked = false;

	int find(const TCHAR *fn);
	int popFirstAvailableID();
	void setAvailable(int id);
};

